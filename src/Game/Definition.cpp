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

#include <cstring>
#include <cstdio>
#include <map>
#include <string>
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
    DEF_FIELD_IMAGE_PAIR = 11
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
    { "FieldType", 0, DEF_FIELD_ENUM, nullptr },
    { "x", 4, DEF_FIELD_TRACK, nullptr },
    { "y", 12, DEF_FIELD_TRACK, nullptr },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gTransformDefFields[] = {
    { "x", 0, DEF_FIELD_FLOAT, nullptr },
    { "y", 4, DEF_FIELD_FLOAT, nullptr },
    { "kx", 8, DEF_FIELD_FLOAT, nullptr },
    { "ky", 12, DEF_FIELD_FLOAT, nullptr },
    { "sx", 16, DEF_FIELD_FLOAT, nullptr },
    { "sy", 20, DEF_FIELD_FLOAT, nullptr },
    { "f", 24, DEF_FIELD_FLOAT, nullptr },
    { "a", 28, DEF_FIELD_FLOAT, nullptr },
    { "i", 32, DEF_FIELD_IMAGE_PAIR, nullptr },
    { "font", 40, DEF_FIELD_FONT, nullptr },
    { "text", 44, DEF_FIELD_STRING, nullptr },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gTrackDefFields[] = {
    { "name", 4, DEF_FIELD_STRING, nullptr },
    { "t", 8, DEF_FIELD_ARRAY, &gTransformDefMap },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gReanimatorDefFields[] = {
    { "track", 0, DEF_FIELD_ARRAY, &gTrackDefMap },
    { "fps", 8, DEF_FIELD_FLOAT, nullptr },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gEmitterDefFields[] = {
    { "Image", 0, DEF_FIELD_IMAGE, nullptr },
    { "ImageRow", 8, DEF_FIELD_INT, nullptr },
    { "ImageCol", 4, DEF_FIELD_INT, nullptr },
    { "ImageFrames", 12, DEF_FIELD_INT, nullptr },
    { "Animated", 16, DEF_FIELD_INT, nullptr },
    { "ParticleFlags", 20, DEF_FIELD_FLAGS, nullptr },
    { "EmitterType", 24, DEF_FIELD_ENUM, nullptr },
    { "Name", 28, DEF_FIELD_STRING, nullptr },
    { "SystemDuration", 36, DEF_FIELD_TRACK, nullptr },
    { "OnDuration", 32, DEF_FIELD_STRING, nullptr },
    { "CrossFadeDuration", 44, DEF_FIELD_TRACK, nullptr },
    { "SpawnRate", 52, DEF_FIELD_TRACK, nullptr },
    { "SpawnMinActive", 60, DEF_FIELD_TRACK, nullptr },
    { "SpawnMaxActive", 68, DEF_FIELD_TRACK, nullptr },
    { "SpawnMaxLaunched", 76, DEF_FIELD_TRACK, nullptr },
    { "EmitterRadius", 84, DEF_FIELD_TRACK, nullptr },
    { "EmitterOffsetX", 92, DEF_FIELD_TRACK, nullptr },
    { "EmitterOffsetY", 100, DEF_FIELD_TRACK, nullptr },
    { "EmitterBoxX", 108, DEF_FIELD_TRACK, nullptr },
    { "EmitterBoxY", 116, DEF_FIELD_TRACK, nullptr },
    { "EmitterPath", 140, DEF_FIELD_TRACK, nullptr },
    { "EmitterSkewX", 124, DEF_FIELD_TRACK, nullptr },
    { "EmitterSkewY", 132, DEF_FIELD_TRACK, nullptr },
    { "ParticleDuration", 148, DEF_FIELD_TRACK, nullptr },
    { "SystemRed", 172, DEF_FIELD_TRACK, nullptr },
    { "SystemGreen", 180, DEF_FIELD_TRACK, nullptr },
    { "SystemBlue", 188, DEF_FIELD_TRACK, nullptr },
    { "SystemAlpha", 196, DEF_FIELD_TRACK, nullptr },
    { "SystemBrightness", 204, DEF_FIELD_TRACK, nullptr },
    { "LaunchSpeed", 156, DEF_FIELD_TRACK, nullptr },
    { "LaunchAngle", 164, DEF_FIELD_TRACK, nullptr },
    { "Field", 212, DEF_FIELD_ARRAY, &gParticleFieldDefMap },
    { "SystemField", 220, DEF_FIELD_ARRAY, &gParticleFieldDefMap },
    { "ParticleRed", 228, DEF_FIELD_TRACK, nullptr },
    { "ParticleGreen", 236, DEF_FIELD_TRACK, nullptr },
    { "ParticleBlue", 244, DEF_FIELD_TRACK, nullptr },
    { "ParticleAlpha", 252, DEF_FIELD_TRACK, nullptr },
    { "ParticleBrightness", 260, DEF_FIELD_TRACK, nullptr },
    { "ParticleSpinAngle", 268, DEF_FIELD_TRACK, nullptr },
    { "ParticleSpinSpeed", 276, DEF_FIELD_TRACK, nullptr },
    { "ParticleScale", 284, DEF_FIELD_TRACK, nullptr },
    { "ParticleStretch", 292, DEF_FIELD_TRACK, nullptr },
    { "CollisionReflect", 300, DEF_FIELD_TRACK, nullptr },
    { "CollisionSpin", 308, DEF_FIELD_TRACK, nullptr },
    { "ClipTop", 316, DEF_FIELD_TRACK, nullptr },
    { "ClipBottom", 324, DEF_FIELD_TRACK, nullptr },
    { "ClipLeft", 332, DEF_FIELD_TRACK, nullptr },
    { "ClipRight", 340, DEF_FIELD_TRACK, nullptr },
    { "AnimationRate", 348, DEF_FIELD_TRACK, nullptr },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gParticleDefFields[] = {
    { "Emitter", 0, DEF_FIELD_ARRAY, &gEmitterDefMap },
    { gEmptyCString, 0, 0, nullptr }
};

static const DefField gTrailDefFields[] = {
    { "Image", 0, DEF_FIELD_IMAGE, nullptr },
    { "MaxPoints", 4, DEF_FIELD_INT, nullptr },
    { "MinPointDistance", 8, DEF_FIELD_FLOAT, nullptr },
    { "TrailFlags", 12, DEF_FIELD_FLAGS, nullptr },
    { "WidthOverLength", 24, DEF_FIELD_TRACK, nullptr },
    { "WidthOverTime", 32, DEF_FIELD_TRACK, nullptr },
    { "AlphaOverLength", 40, DEF_FIELD_TRACK, nullptr },
    { "AlphaOverTime", 48, DEF_FIELD_TRACK, nullptr },
    { "TrailDuration", 16, DEF_FIELD_TRACK, nullptr },
    { gEmptyCString, 0, 0, nullptr }
};

DefMap gParticleDefMap;
DefMap gTrailDefMap;

static void EnsureDefMaps()
{
    if (gParticleDefMap.mFields != nullptr)
        return;
    gParticleFieldDefMap.mFields = gParticleFieldDefFields;
    gParticleFieldDefMap.mSize = 20;
    gTransformDefMap.mFields = gTransformDefFields;
    gTransformDefMap.mSize = 48;
    gTrackDefMap.mFields = gTrackDefFields;
    gTrackDefMap.mSize = 20;
    gEmitterDefMap.mFields = gEmitterDefFields;
    gEmitterDefMap.mSize = 356;
    gReanimatorDefMap.mFields = gReanimatorDefFields;
    gReanimatorDefMap.mSize = 16;
    gParticleDefMap.mFields = gParticleDefFields;
    gParticleDefMap.mSize = 8;
    gTrailDefMap.mFields = gTrailDefFields;
    gTrailDefMap.mSize = 56;
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
        operator delete[](gParticleDefArray);
    gParticleDefArray = nullptr;
    gParticleDefCount = 0;
    gParticleParamArray = nullptr;
    gParticleParamArraySize = 0;
}

static void* DefinitionAlloc(int theSize)
{
    void* aBlock = operator new[]((unsigned)theSize);
    std::memset(aBlock, 0, (unsigned)theSize);
    return aBlock;
}

static void DefinitionFreeBlock(void* theBlock)
{
    if (theBlock != nullptr)
        operator delete[](theBlock);
}

static int ReadCacheInt(const char*& theCursor)
{
    int aValue = 0;
    std::memcpy(&aValue, theCursor, 4);
    theCursor += 4;
    return aValue;
}

static void ReadCacheBytes(const char*& theCursor, void* theDest, int theCount)
{
    if (theCount > 0)
        std::memcpy(theDest, theCursor, (unsigned)theCount);
    theCursor += theCount;
}

static const char* ReadCacheCString(const char*& theCursor)
{
    int aLength = ReadCacheInt(theCursor);
    if (aLength == 0)
        return gEmptyCString;
    char* aText = static_cast<char*>(DefinitionAlloc(aLength + 1));
    ReadCacheBytes(theCursor, aText, aLength);
    aText[aLength] = 0;
    return aText;
}

static int DefMapReadFromCache(const char*& theCursor, const DefMap* theMap, void* theObject);

static int ReadCacheArray(const char*& theCursor, DefinitionArrayField* theField, const DefMap* theNested)
{
    int aCount = ReadCacheInt(theCursor);
    if (aCount != theNested->mSize)
        return 0;
    theField->mCount = aCount;
    if (theField->mCount == 0)
        return 1;
    theField->mData = DefinitionAlloc(theNested->mSize * theField->mCount);
    ReadCacheBytes(theCursor, theField->mData, theNested->mSize * theField->mCount);
    char* aElement = static_cast<char*>(theField->mData);
    for (int aIndex = 0; aIndex < theField->mCount; ++aIndex)
    {
        if (DefMapReadFromCache(theCursor, theNested, aElement) == 0)
            return 0;
        aElement += theNested->mSize;
    }
    return 1;
}

static int ReadCacheTrack(const char*& theCursor, FloatParameterTrack* theTrack)
{
    theTrack->mCountNodes = ReadCacheInt(theCursor);
    if (theTrack->mCountNodes <= 0)
        return 1;
    theTrack->mNodes = static_cast<FloatParameterTrackNode*>(
        DefinitionAlloc(20 * theTrack->mCountNodes));
    ReadCacheBytes(theCursor, theTrack->mNodes, 20 * theTrack->mCountNodes);
    return 1;
}

static int ReadCacheString(const char*& theCursor, const char** theSlot)
{
    *theSlot = ReadCacheCString(theCursor);
    return 1;
}

template <class T>
T* FieldAt(void* theObject, int theOffset)
{
    char* aBytes = (char*)theObject;
    return (T*)(aBytes + theOffset);
}

static int DefMapReadFromCache(const char*& theCursor, const DefMap* theMap, void* theObject)
{
    if (theMap->mFields == nullptr || theMap->mFields->mName == nullptr || theMap->mFields->mName[0] == 0)
        return 1;
    for (const DefField* aField = theMap->mFields; aField->mName != nullptr && aField->mName[0] != 0; ++aField)
    {
        if (aField->mType == DEF_FIELD_STRING)
        {
            if (ReadCacheString(theCursor, FieldAt<const char*>(theObject, aField->mOffset)) == 0)
                return 0;
        }
        else if (aField->mType == DEF_FIELD_ARRAY)
        {
            if (ReadCacheArray(theCursor, FieldAt<DefinitionArrayField>(theObject, aField->mOffset),
                               static_cast<const DefMap*>(aField->mExtra)) == 0)
                return 0;
        }
        else if (aField->mType == DEF_FIELD_TRACK)
        {
            if (ReadCacheTrack(theCursor, FieldAt<FloatParameterTrack>(theObject, aField->mOffset)) == 0)
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
    const char* aCursor = aBytes;
    int aHash = ReadCacheInt(aCursor);
    (void)aHash;
    ReadCacheBytes(aCursor, theObject, theMap->mSize);
    int aOk = DefMapReadFromCache(aCursor, theMap, theObject);
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
    theTrack->mNodes = static_cast<FloatParameterTrackNode*>(DefinitionAlloc(20));
    theTrack->mNodes->mTime = 0.0f;
    theTrack->mNodes->mLowValue = theValue;
    theTrack->mNodes->mHighValue = theValue;
    theTrack->mNodes->mCurveType = (TodCurves)0;
    theTrack->mNodes->mDistribution = (TodCurves)1;
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
    return DefinitionCompileAndLoad(Sexy::String(theFileName), &gParticleDefMap, theDefinition);
}

void TodParticleLoadDefinitions(ParticleParamRecord* theParams, int theCount)
{
    TodHesitationBracket aBracket("TodParticleLoadDefinitions");
    gParticleParamArraySize = theCount;
    gParticleParamArray = theParams;
    gParticleDefCount = theCount;
    gParticleDefArray = static_cast<TodParticleDefinition*>(
        operator new[]((unsigned)(8 * theCount)));
    for (int aIndex = 0; aIndex < theCount; ++aIndex)
        ConstructRaw<TodParticleDefinition>(&gParticleDefArray[aIndex]);
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

static void FillMissingFloat(float& thePrevious, float& theValue)
{
    if (theValue == MissingSentinel())
        theValue = thePrevious;
    else
        thePrevious = theValue;
}

int ReanimationLoadDefinition(const Sexy::String& theFileName, ReanimatorDefinition* theDefinition)
{
    EnsureDefMaps();
    int aLoaded = DefinitionCompileAndLoad(theFileName, &gReanimatorDefMap, theDefinition);
    if (aLoaded == 0)
        return 0;
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
        char aMessage[1024];
        snprintf(aMessage, sizeof(aMessage), "Failed to load reanim '%s'", aParam->mFileName);
        TodErrorMessageBox(aMessage, "Error");
    }
    for (int aTrack = 0; aTrack < aDefinition->mTrackCount; ++aTrack)
    {
        ReanimatorTrack& aRow = aDefinition->mTracks[aTrack];
        aRow.mTrackId = ReanimatorTrackNameToId(aRow.mName);
        aRow.mIsAttacher = _strnicmp(aRow.mName, "attacher__", 10) == 0;
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
        operator new[]((unsigned)(16 * theCount)));
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
