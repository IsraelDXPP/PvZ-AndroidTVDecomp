// Sexy::Board -- the framework demo's effect viewer, not the game's Board.
//
// Every method is a `_ZN4Sexy5Board*` symbol. The widget is dead in the shipped
// binary (nothing constructs it), but the bodies are a complete particle / trail
// / reanimation viewer: load a definition, spawn it on click, and dump buddy
// icons of the current reanimation.

#include "Board.h"
#include "Definition.h"
#include "EffectSystem.h"
#include "Font.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "Image.h"
#include "ImageLib.h"
#include "MemoryImage.h"
#include "PerfTimer.h"
#include "ReanimAtlas.h"
#include "Reanimation.h"
#include "ReanimationHolder.h"
#include "ReanimatorDefinition.h"
#include "ReanimatorTransform.h"
#include "ReconData.h"
#include "ReconExterns.h"
#include "ReconFreeFunctions.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"
#include "SharedImageRef.h"
#include "TodParticleDefinition.h"
#include "TodParticleHolder.h"
#include "TodParticleSystem.h"
#include "Trail.h"
#include "TrailDefinition.h"
#include "TrailHolder.h"
#include "WidgetManager.h"
#include <cmath>
#include <cstring>
#include <new>

namespace Sexy
{
bool FileExists(const Sexy::String &thePath);
}
namespace Sexy
{
int GetFileDate(const Sexy::String &thePath);
}

#include "ResourceManager.h"

namespace
{

const int kMaxImageNames = 100;
const int kMaxReanimDefs = 100;
const int kBuddyFrames = 32;
const int kBuddyCanvas = 512;
const float kBuddyDrawScale = 1.5f;
const float kBuddyFrameStep = 0.033333f;
const unsigned int kMemoryImageCheck = 1273952990u;

// DataArrayItem is { T, uint32 id }. The id's high halfword is zero while the
// slot is on the free list, which is the "in use" test every pool walk uses.
bool SlotInUse(unsigned int theId)
{
    return (theId >> 16) != 0;
}

ParticleSystemSlot *ParticleSlots(TodParticleHolder *theHolder)
{
    return theHolder->mParticleSystems.mBlock;
}

TrailSlot *TrailSlots(TrailHolder *theHolder)
{
    return theHolder->mTrails.mBlock;
}

ReanimationSlot *ReanimSlots(ReanimationHolder *theHolder)
{
    return theHolder->mBlock;
}

// A shared copy-on-write rep (refcount >= 0) is detached before the name is
// read. The loader does this on the image's file-name string, then tests the
// first character.
void DetachImageName(Sexy::String *theName)
{
    // A shared rep (refcount >= 0) is detached before the first character is
    // tested. StringDisposeLeaked is that detach; an empty name has nothing
    // to detach.
    if (theName->empty())
        return;
    StringDisposeLeaked(theName);
}

bool ImageHasFileName(Sexy::Image *theImage)
{
    DetachImageName(&theImage->mFileName);
    return !theImage->mFileName.empty();
}

const char *BaseNameOf(const char *thePath)
{
    const char *aSlash = strrchr(thePath, '/');
    const char *aBack = strrchr(thePath, '\\');
    const char *aCut = aSlash;
    if (aBack != nullptr && (aCut == nullptr || aBack > aCut))
        aCut = aBack;
    if (aCut == nullptr)
        return thePath;
    return aCut + 1;
}

// The binary reads the current animation time as word 5 of the reanimation.
// That word is mAnimTime; GetPosition reads the same overlay matrix the buddy
// code saves and restores around the off-screen draw.
float AnimTimeOf(Reanimation *theReanim)
{
    return theReanim->mAnimTime;
}

void SetAnimTime(Reanimation *theReanim, float theTime)
{
    theReanim->mAnimTime = theTime;
}

// A 512x512 memory image whose pixel buffer carries the framework's check word
// one past the last pixel. FindBuddyImageSize and WritePNGImage both read it.
MemoryImage *MakeBuddyCanvas(int theWidth, int theHeight)
{
    MemoryImage *aCanvas = new MemoryImage();
    aCanvas->mWidth = theWidth;
    aCanvas->mHeight = theHeight;
    unsigned int aPixels = (unsigned int)theWidth * (unsigned int)theHeight;
    std::uint32_t *aBits = new std::uint32_t[aPixels + 1];
    memset(aBits, 0, aPixels * sizeof(*aBits));
    aBits[aPixels] = kMemoryImageCheck;
    aCanvas->mBits = aBits;
    aCanvas->mPurgeBits = true;
    aCanvas->mBitsChanged = true;
    return aCanvas;
}

void ClearCanvas(MemoryImage *theCanvas)
{
    unsigned int aPixels = (unsigned int)theCanvas->mWidth * (unsigned int)theCanvas->mHeight;
    if (theCanvas->mBits != nullptr)
        memset(theCanvas->mBits, 0, aPixels * sizeof(*theCanvas->mBits));
}

// ImageLib::Image stores the same pixel words as unsigned int. The two
// pointers name one buffer; the PNG writer does not take ownership.
struct BuddyPng : ImageLib::Image
{
    explicit BuddyPng(MemoryImage *theCanvas)
    {
        mWidth = theCanvas->mWidth;
        mHeight = theCanvas->mHeight;
        mBits = theCanvas->mBits;
    }
    ~BuddyPng()
    {
        mBits = nullptr;
    }
};

} // namespace

// ==========================================================================
// Board::AddedToManager
// @001F5F20
// ==========================================================================
void Sexy::Board::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    WidgetContainer::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mLoadNewEffectButton);
}

// ==========================================================================
// Board::RemovedFromManager
// @001F5F38
// ==========================================================================
void Sexy::Board::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    WidgetContainer::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mLoadNewEffectButton);
}

// ==========================================================================
// Board::ButtonPress
// @001F5F14  empty; ButtonDepress is what acts on the id.
// ==========================================================================
void Sexy::Board::ButtonPress(int theId)
{
    (void)theId;
}

// ==========================================================================
// Board::Update
// @001F8AAC
// ==========================================================================
void Sexy::Board::Update()
{
    Widget::Update();
    MarkDirty();

    // Live trails get a point on a circle of radius 150 centred at (300, 300).
    mFrameCounter++;
    float aAngle = (float)mFrameCounter * -0.033333f;
    float aX = sinf(aAngle) * 150.0f + 300.0f;
    float aY = cosf(aAngle) * 150.0f + 300.0f;

    TrailHolder *aTrails = mEffectSystem->mTrailHolder;
    for (unsigned int i = 0; i < aTrails->mTrails.mMaxUsedCount; ++i)
    {
        TrailSlot *aSlot = &TrailSlots(aTrails)[i];
        if (aSlot->mItem.mDead)
            continue;
        aSlot->mItem.AddPoint(aX, aY);
    }

    mEffectSystem->Update();
    mEffectSystem->ProcessDeleteQueue();
}

// ==========================================================================
// Board::KeyChar
// @001F919C
// ==========================================================================
int Sexy::Board::KeyChar(char theChar)
{
    switch (theChar)
    {
        case ' ':
            LoadEffectDef();
            break;
        case 'T':
            DoReanimatorTest();
            break;
        case 'b':
            Blink();
            break;
        case '+':
            NextAnim();
            break;
        case 'f':
            FadeOutParticles();
            break;
        case 'o':
            MakeBuddyImage();
            break;
        case 's':
            MakeBuddyImageStrip();
            break;
        case 'a':
            MakeAllBuddyImages();
            break;
        case '%':
            mApp->SwitchScreenModeFull(mApp->mWindowed, !mApp->Is3DAccelerated(), false);
            break;
        default:
            break;
    }
    return 1;
}

// ==========================================================================
// Board::Draw
// @001F784C
// ==========================================================================
void Sexy::Board::Draw(Sexy::Graphics *g)
{
    if (!mApp->mLoadingThreadCompleted)
    {
        g->SetColor(Color(64, 64, 64));
        g->FillRect(0, 0, mWidth, mHeight);
        return;
    }

    SharedImageRef aBackground = mApp->mResourceManager->LoadImage("IMAGE_BACKGROUND2");
    if (aBackground.mImage != nullptr)
        g->DrawImage(aBackground.mImage, -220, 0);
    else
    {
        g->SetColor(Color(255, 255, 255));
        g->FillRect(0, 0, mWidth, mHeight);
    }

    // The definition's atlas carries its packed preview image at +0x504.
    if (mReanimatorDef != nullptr && mReanimatorDef->mReanimAtlas != nullptr)
    {
        Sexy::Image *aPreview = mReanimatorDef->mReanimAtlas->mImage;
        if (aPreview != nullptr)
            g->DrawImage(aPreview, 100, 100);
    }

    // Caption is the file's base name with the extension stripped, then the
    // full track name appended. Drawn at (5, 15) in white.
    Sexy::String aCaption = mTrackName;
    const char *aPath = aCaption.c_str();
    const char *aBase = BaseNameOf(aPath);
    if (aBase != aPath)
        aCaption = aCaption.substr((unsigned int)(aBase - aPath));

    const char *aName = aCaption.c_str();
    const char *aDot = strrchr(aName, '.');
    if (aDot != nullptr)
        aCaption = aCaption.substr(0, (unsigned int)(aDot - aName));

    Sexy::String aText;
    StrFormat(&aText, "%s%s", aCaption.c_str(), mTrackName.c_str());
    g->SetFont(mFont);
    g->SetColor(Color(255, 255, 255));
    g->DrawString(aText, 5, 15);

    TodParticleHolder *aParticles = mEffectSystem->mParticleHolder;
    for (unsigned int i = 0; i < aParticles->mParticleSystems.mMaxUsedCount; ++i)
    {
        ParticleSystemSlot *aSlot = &ParticleSlots(aParticles)[i];
        if (!SlotInUse(aSlot->mID))
            continue;
        aSlot->mItem.Draw(g);
    }

    TrailHolder *aTrails = mEffectSystem->mTrailHolder;
    for (unsigned int i = 0; i < aTrails->mTrails.mMaxUsedCount; ++i)
    {
        TrailSlot *aSlot = &TrailSlots(aTrails)[i];
        if (!SlotInUse(aSlot->mID))
            continue;
        aSlot->mItem.Draw(g);
    }

    ReanimationHolder *aReanims = mEffectSystem->mReanimationHolder;
    for (unsigned int i = 0; i < aReanims->mMaxUsedCount; ++i)
    {
        ReanimationSlot *aSlot = &ReanimSlots(aReanims)[i];
        if (!SlotInUse(aSlot->mID))
            continue;
        if (aSlot->mItem.mIsAttachment)
            continue;
        aSlot->mItem.Draw(g);
    }

    g->SetColor(Color(128, 128, 128, 255));
    g->FillRect(0, 0, 200, 50);
    DrawPerfText(g);
}

// ==========================================================================
// Board::MouseDown
// @001F8318
// ==========================================================================
void Sexy::Board::MouseDown(int x, int y, int theClickCount)
{
    (void)theClickCount;

    bool aUseLoaded = false;
    if (mParticleDef != nullptr || mTrailDef != nullptr || mReanimatorDef != nullptr)
        aUseLoaded = !DefinitionHasChanged();
    if (!aUseLoaded)
        LoadEffectDef();

    if (mParticleDef != nullptr)
    {
        mEffectSystem->mParticleHolder->AllocParticleSystemFromDef(
            (float)x, (float)y, 0, mParticleDef, (ParticleEffect)0);
        return;
    }

    if (mTrailDef != nullptr)
    {
        Trail *aTrail = mEffectSystem->mTrailHolder->AllocTrailFromDef(0, mTrailDef);
        aTrail->mOffsetX = (float)x;
        aTrail->mOffsetY = (float)y;
        return;
    }

    if (mReanimatorDef == nullptr)
        return;

    Reanimation *aReanim =
        mEffectSystem->mReanimationHolder->AllocReanimation((float)x, (float)y, 0, REANIM_LOADBAR_SPROUT);
    aReanim->SetAnimRate(0.0f);

    if (aReanim->TrackExists("anim_idle"))
    {
        aReanim->SetFramesForLayer("anim_idle");
        mTrackName = "anim_idle";
        aReanim->mFrameBasePose = aReanim->mFrameCount;
    }
    else
    {
        mTrackName = "no track";
    }

    aReanim->AssignRenderGroupToTrack("Cornpult_butter", -1);
    aReanim->AssignRenderGroupToTrack("anim_waterline", -1);

    if (!aReanim->TrackExists("anim_head_idle"))
        return;

    Reanimation *aHead =
        mEffectSystem->mReanimationHolder->AllocReanimation((float)x, (float)y, 0, REANIM_LOADBAR_SPROUT);
    aHead->SetAnimRate(0.0f);
    aHead->mFrameStart = aReanim->mFrameStart;
    aHead->SetFramesForLayer("anim_head_idle");

    if (aReanim->TrackExists("anim_stem"))
        aHead->AttachToAnotherReanimation(aReanim, "anim_stem");
    else if (aReanim->TrackExists("anim_idle"))
        aHead->AttachToAnotherReanimation(aReanim, "anim_idle");
}

// ==========================================================================
// Board::~Board
// @001F6B14
// ==========================================================================
Sexy::Board::~Board()
{
    mEffectSystem->EffectSystemDispose();
    ClearDefs();
    delete mEffectSystem;

    RemoveAllWidgets(false, false, nullptr);
    mLoadNewEffectButton->mWidgetManager = nullptr;
    delete mLoadNewEffectButton;
    delete mFont;
}

// ==========================================================================
// Board::MakeFullImageFileName
// @001F5F50
//
// The parameter is passed by value, so a successful rewrite never reaches the
// caller. The return is what the loaders test.
// ==========================================================================
int Sexy::Board::MakeFullImageFileName(Sexy::String theFileName)
{
    if (Sexy::FileExists(theFileName))
        return 1;

    static const char *kExtensions[6] = {".png", ".tga", ".jpg", ".gif", ".j2k", ".jp2"};
    for (int i = 0; i < 6; ++i)
    {
        Sexy::String aCandidate = theFileName;
        aCandidate += kExtensions[i];
        if (Sexy::FileExists(aCandidate))
        {
            theFileName = aCandidate;
            return 1;
        }
    }

    TodErrorMessageBox("Unknown image type", "Error");
    return 0;
}

// ==========================================================================
// Board::TryToLoadParticle
// @001F635C
// ==========================================================================
int Sexy::Board::TryToLoadParticle()
{
    int aLoaded = TodParticleLoadADef(mParticleDef, mFileName.c_str());
    if (aLoaded == 0)
    {
        TodErrorMessageBox("Failed to load particle xml", "Error");
        return 0;
    }

    mNumImages = 0;
    TodEmitterDefinition *aEmitters = mParticleDef->mEmitterDefs;
    for (int i = 0; i < mParticleDef->mEmitterDefCount; ++i)
    {
        TodEmitterDefinition *aEmitter = &aEmitters[i];
        Sexy::Image *aImage = aEmitter->mImage;
        if (aImage != nullptr)
        {
            int aColSpan = aEmitter->mImageCol + aEmitter->mImageFrames;
            if (aEmitter->mImageRow >= aImage->mNumCols || aColSpan > aImage->mNumRows)
            {
                TodErrorMessageBox("Invalid row/col in particle xml", "Error");
                return 0;
            }
            if (ImageHasFileName(aImage))
            {
                mImageNames[mNumImages] = aImage->mFileName;
                ++mNumImages;
                if (mNumImages == kMaxImageNames)
                    return aLoaded;
            }
        }
        else if (!FloatTrackIsSet(&aEmitter->mEmitterOffsetX))
        {
            TodErrorMessageBox("Invalid image in particle xml", "Error");
            return 0;
        }
    }
    return aLoaded;
}

// ==========================================================================
// Board::TryToLoadTrail
// @001F6454
// ==========================================================================
int Sexy::Board::TryToLoadTrail()
{
    int aLoaded = TrailLoadADef(mTrailDef, mFileName.c_str());
    if (aLoaded == 0)
    {
        TodErrorMessageBox("Failed to load trail file", "Error");
        return 0;
    }

    Sexy::Image *aImage = mTrailDef->mImage;
    if (aImage == nullptr)
    {
        TodErrorMessageBox("Invalid image in trail xml", "Error");
        return 0;
    }

    if (ImageHasFileName(aImage))
        return aLoaded;

    mImageNames[0] = aImage->mFileName;
    mNumImages = 1;
    return aLoaded;
}

// ==========================================================================
// Board::TryToLoadReanimation
// @001F64D8
// ==========================================================================
int Sexy::Board::TryToLoadReanimation()
{
    ReanimatorDefinition *aDefs = new ReanimatorDefinition[kMaxReanimDefs];
    mReanimatorDef = aDefs;
    gReanimatorDefArray = aDefs;
    gReanimatorDefCount = 1;

    int aLoaded = ReanimationLoadDefinition(mFileName, aDefs);
    if (aLoaded == 0)
    {
        TodErrorMessageBox("Failed to load reanim file", "Error");
        return 0;
    }

    gReanimationParamArray = new ReanimationParamRecord[kMaxReanimDefs];
    gReanimationParamArray[0].mReanimationType = REANIM_LOADBAR_SPROUT;
    Sexy::String *aKeptName = new Sexy::String(mFileName);
    gReanimationParamArray[0].mFileName = aKeptName->c_str();
    gReanimationParamArraySize = 1;
    mNumImages = 0;

    if (mReanimatorDef->mTrackCount <= 0)
        return aLoaded;

    // A definition with no atlas lists every transform image. An atlas skips
    // that list and goes straight to the attacher walk below.
    if (mReanimatorDef->mReanimAtlas == nullptr)
    {
        for (int aTrack = 0; aTrack < mReanimatorDef->mTrackCount; ++aTrack)
        {
            ReanimatorTrack &aRow = mReanimatorDef->mTracks[aTrack];
            for (int aFrame = 0; aFrame < aRow.mTransformCount; ++aFrame)
            {
                Sexy::Image *aImage = aRow.mTransforms[aFrame].mImage;
                if (aImage == nullptr)
                    continue;
                if (!ImageHasFileName(aImage))
                    continue;
                mImageNames[mNumImages] = aImage->mFileName;
                ++mNumImages;
                if (mNumImages == kMaxImageNames)
                    break;
            }
            if (mNumImages == kMaxImageNames)
                break;
        }
        if (mReanimatorDef->mTrackCount <= 0)
            return aLoaded;
    }

    for (int aTrack = 0; aTrack < mReanimatorDef->mTrackCount; ++aTrack)
    {
        ReanimatorTrack &aRow = mReanimatorDef->mTracks[aTrack];
        if (strncasecmp(aRow.mName, "attacher__", 10) != 0)
            continue;
        if (aRow.mTransformCount <= 0)
            continue;

        for (int aFrame = 0; aFrame < aRow.mTransformCount; ++aFrame)
        {
            ReanimatorTransform &aTransform = aRow.mTransforms[aFrame];
            if (aTransform.mText == nullptr || aTransform.mText[0] == 0)
                continue;

            AttacherInfo aInfo;
            Reanimation aParser;
            aParser.ParseAttacherTrack(aTransform, aInfo);
            if (aInfo.mReanimName.empty())
                continue;

            Sexy::String aFileName;
            StrFormat(&aFileName, "reanim/%s.reanim", aInfo.mReanimName.c_str());

            bool aAlreadyLoaded = false;
            for (int i = 0; i < gReanimationParamArraySize; ++i)
            {
                if (strcasecmp(aFileName.c_str(), gReanimationParamArray[i].mFileName) == 0)
                {
                    aAlreadyLoaded = true;
                    break;
                }
            }
            if (aAlreadyLoaded)
                continue;

            if (ReanimationLoadDefinition(aFileName, &gReanimatorDefArray[gReanimatorDefCount]) == 0)
            {
                TodErrorMessageBox("Failed to load attach reanim file", "Error");
                return 0;
            }

            int aIndex = gReanimationParamArraySize;
            gReanimationParamArray[aIndex].mReanimationType = (ReanimationType)aIndex;
            Sexy::String *aAttachName = new Sexy::String(aFileName);
            gReanimationParamArray[aIndex].mFileName = aAttachName->c_str();
            gReanimationParamArraySize = aIndex + 1;
            ++gReanimatorDefCount;
            if (aIndex + 1 > 99 || gReanimatorDefCount > 99)
            {
                TodErrorMessageBox("Too many attachments", "Error");
                return 0;
            }
        }
    }
    return aLoaded;
}

// ==========================================================================
// Board::ClearDefs
// @001F6A40
// ==========================================================================
void Sexy::Board::ClearDefs()
{
    if (mParticleDef != nullptr)
    {
        DefinitionFreeMap(&gParticleDefMap, mParticleDef);
        operator delete(mParticleDef);
        mParticleDef = nullptr;
    }
    if (mTrailDef != nullptr)
    {
        DefinitionFreeMap(&gTrailDefMap, mTrailDef);
        mTrailDef->~TrailDefinition();
        operator delete(mTrailDef);
        mTrailDef = nullptr;
    }
    if (mReanimatorDef != nullptr)
    {
        for (int i = 0; i < gReanimatorDefCount; ++i)
            ReanimationFreeDefinition(&gReanimatorDefArray[i]);

        if (gReanimatorDefArray != nullptr)
            operator delete[](gReanimatorDefArray);
        gReanimatorDefArray = nullptr;
        gReanimatorDefCount = 0;
        mReanimatorDef = nullptr;

        delete[] gReanimationParamArray;
        gReanimationParamArray = nullptr;
        gReanimationParamArraySize = 0;
    }
}

// ==========================================================================
// Board::TryToLoadEffect
// @001F6CC8
// ==========================================================================
void Sexy::Board::TryToLoadEffect()
{
    TodParticleFreeDefinitions();
    TodParticleLoadDefinitions(nullptr, 0);
    TrailFreeDefinitions();
    TrailLoadDefinitions(nullptr, 0);
    gReanimatorDefCount = 0;
    gReanimatorDefArray = nullptr;
    ReanimatorLoadDefinitions(nullptr, 0);

    // The old manager is thrown away the same way LawnApp replaces the one the
    // base constructed. ResourceManager has no virtual shutdown of its own.
    delete mApp->mResourceManager;
    mApp->mResourceManager = new Sexy::ResourceManager(mApp);
    mApp->LoadResourceManifest();
    mApp->CleanSharedImages();

    const char *aFileName = mFileName.c_str();
    if (strstr(aFileName, ".xml") != nullptr)
    {
        mParticleDef = new TodParticleDefinition();
        if (TryToLoadParticle() == 0)
        {
            delete mParticleDef;
            mParticleDef = nullptr;
            return;
        }
    }
    else if (strstr(aFileName, ".trail") != nullptr)
    {
        mTrailDef = new TrailDefinition();
        if (TryToLoadTrail() == 0)
        {
            delete mTrailDef;
            mTrailDef = nullptr;
            return;
        }
    }
    else if (strstr(aFileName, ".reanim") != nullptr)
    {
        if (TryToLoadReanimation() == 0)
        {
            ClearDefs();
            return;
        }
    }
    else
    {
        TodErrorMessageBox("Unknown effect file type", "Error");
        return;
    }

    for (int i = 0; i < mNumImages; ++i)
    {
        Sexy::String aCopy(mImageNames[i]);
        MakeFullImageFileName(aCopy);
        mFileDates[1 + i] = Sexy::GetFileDate(mImageNames[i]);
    }
    mFileDates[0] = Sexy::GetFileDate(mFileName);
    mResourceDate = Sexy::GetFileDate(Sexy::String("properties/resources.xml"));
}

// ==========================================================================
// Board::ClearEffects
// @001F6F0C
// ==========================================================================
int Sexy::Board::ClearEffects()
{
    mEffectSystem->EffectSystemDispose();
    return mEffectSystem->EffectSystemInitialize();
}

// ==========================================================================
// Board::LoadEffectDef
// @001F6F24
// ==========================================================================
void Sexy::Board::LoadEffectDef()
{
    ClearEffects();
    ClearDefs();
    if (!mFileName.empty())
        TryToLoadEffect();
}

// ==========================================================================
// Board::Board
// @001F6F48
// ==========================================================================
Sexy::Board::Board(Sexy::SexyAppBase *theApp)
{
    mApp = theApp;
    mParticleDef = nullptr;
    mTrailDef = nullptr;
    mReanimatorDef = nullptr;
    mApp->RegistryReadString("LastFileName", &mFileName);

    mFrameCounter = 0;
    mEffectSystem = new EffectSystem();
    mEffectSystem->EffectSystemInitialize();
    LoadEffectDef();

    // Nothing in the binary assigns mFont. The button still reads it, which is
    // consistent with the class never being constructed in the shipped build.
    mFont = nullptr;
    mLoadNewEffectButton = new HyperlinkWidget(0, this);
    mLoadNewEffectButton->mLabel = "Load New Effect...";
    mLoadNewEffectButton->mFont = mFont;
    mLoadNewEffectButton->mColor = Sexy::Color(255);
    mLoadNewEffectButton->mOverColor = Sexy::Color(0x4040FF);
    mLoadNewEffectButton->Resize(5, 18, mFont->StringWidth(mLoadNewEffectButton->mLabel), mFont->GetHeight());
}

// ==========================================================================
// Board::DrawPerfText
// @001F7358
// ==========================================================================
void Sexy::Board::DrawPerfText(Sexy::Graphics *g)
{
    Sexy::String aText;

    if (mParticleDef != nullptr)
    {
        int aSystems = 0;
        int aEmitters = 0;
        int aParticles = 0;
        int aPixels = 0;

        TodParticleHolder *aHolder = mEffectSystem->mParticleHolder;
        for (unsigned int i = 0; i < aHolder->mParticleSystems.mMaxUsedCount; ++i)
        {
            ParticleSystemSlot *aSlot = &ParticleSlots(aHolder)[i];
            if (!SlotInUse(aSlot->mID))
                continue;
            ++aSystems;
            CalcParticlePerfInfo(&aSlot->mItem, &aEmitters, &aParticles, &aPixels);
        }

        Sexy::String aLine;
        StrFormat(&aLine, "Systems %d\n", aSystems);
        aText += aLine;
        StrFormat(&aLine, "Emitters %d\n", aEmitters);
        aText += aLine;
        StrFormat(&aLine, "Particles %d\n", aParticles);
        aText += aLine;
        StrFormat(&aLine, "Pixels %g (1000's)\n", (float)aPixels * 0.001f);
        aText += aLine;
    }
    else if (mTrailDef != nullptr)
    {
        Sexy::String aLine;
        StrFormat(&aLine, "Trails %d\n", mEffectSystem->mTrailHolder->mTrails.mSize);
        aText += aLine;
    }
    else if (mReanimatorDef != nullptr)
    {
        int aImages = 0;
        int aPixels = 0;
        int aDrawn = 0;

        ReanimationHolder *aHolder = mEffectSystem->mReanimationHolder;
        for (unsigned int i = 0; i < aHolder->mMaxUsedCount; ++i)
        {
            ReanimationSlot *aSlot = &ReanimSlots(aHolder)[i];
            if (!SlotInUse(aSlot->mID))
                continue;
            int aImageCount = 0;
            int aPixelCount = 0;
            CalcReanimPerfInfo(&aSlot->mItem, &aImageCount, &aPixelCount);
            aImages += aImageCount;
            aPixels += aPixelCount;
            if (!aSlot->mItem.mIsAttachment)
                ++aDrawn;
        }

        Sexy::String aLine;
        StrFormat(&aLine, "Reanims %d\n", aDrawn);
        aText += aLine;
        StrFormat(&aLine, "Images %d\n", aImages);
        aText += aLine;
        StrFormat(&aLine, "Pixels %g (1000's)\n", (float)aPixels * 0.001f);
        aText += aLine;
        StrFormat(&aLine, "Track: %s\n", mTrackName.c_str());
        aText += aLine;
    }

    g->SetFont(mFont);
    g->SetColor(Sexy::Color(255, 0, 255));
    g->DrawStringWordWrapped(aText, 10, 120, 10000000, -1, -1, 0);
}

// ==========================================================================
// Board::DefinitionHasChanged
// @001F8258
// ==========================================================================
bool Sexy::Board::DefinitionHasChanged()
{
    if (Sexy::GetFileDate(mFileName) > mFileDates[0])
        return true;

    for (int i = 0; i < mNumImages; ++i)
    {
        if (Sexy::GetFileDate(mImageNames[i]) > mFileDates[1 + i])
            return true;
    }
    return Sexy::GetFileDate(Sexy::String("properties/resources.xml")) > mResourceDate;
}

// ==========================================================================
// Board::NextAnim
// @001F84E0
//
// Advance the displayed track to the next "anim_" layer of the first root
// reanimation, then apply that layer to every root reanimation.
// ==========================================================================
void Sexy::Board::NextAnim()
{
    ReanimationHolder *aHolder = mEffectSystem->mReanimationHolder;

    for (unsigned int i = 0; i < aHolder->mMaxUsedCount; ++i)
    {
        ReanimationSlot *aSlot = &ReanimSlots(aHolder)[i];
        if (!SlotInUse(aSlot->mID))
            continue;
        Reanimation *aReanim = &aSlot->mItem;
        if (aReanim->mIsAttachment)
            continue;
        if (aReanim->mDefinition == nullptr || aReanim->mDefinition->mTrackCount <= 0)
            continue;

        int aFirstAnim = -1;
        int aChosen = -1;
        bool aPassedCurrent = false;
        for (int aTrack = 0; aTrack < aReanim->mDefinition->mTrackCount; ++aTrack)
        {
            const char *aName = aReanim->mDefinition->mTracks[aTrack].mName;
            if (StringEqualCStr(&mTrackName, aName) != 0)
            {
                if (strncasecmp(aName, "anim_", 5) == 0)
                {
                    if (aPassedCurrent)
                    {
                        aChosen = aTrack;
                        break;
                    }
                    if (aFirstAnim == -1)
                        aFirstAnim = aTrack;
                }
            }
            else
            {
                aPassedCurrent = true;
            }
        }
        if (aChosen == -1)
            aChosen = aFirstAnim;
        if (aChosen == -1)
            continue;

        const char *aName = aReanim->mDefinition->mTracks[aChosen].mName;
        mTrackName.assign(aName, (unsigned int)strlen(aName));
        break;
    }

    for (unsigned int i = 0; i < aHolder->mMaxUsedCount; ++i)
    {
        ReanimationSlot *aSlot = &ReanimSlots(aHolder)[i];
        if (!SlotInUse(aSlot->mID))
            continue;
        if (aSlot->mItem.mIsAttachment)
            continue;
        aSlot->mItem.SetFramesForLayer(mTrackName.c_str());
    }
}

// ==========================================================================
// Board::Blink
// @001F8618
//
// Spawn a one-shot blink reanimation on every root reanimation that has a
// blink track, attached to the face when there is one.
// ==========================================================================
int Sexy::Board::Blink()
{
    int aResult = 0;
    ReanimationHolder *aHolder = mEffectSystem->mReanimationHolder;

    for (unsigned int i = 0; i < aHolder->mMaxUsedCount; ++i)
    {
        ReanimationSlot *aBodySlot = &ReanimSlots(aHolder)[i];
        if (!SlotInUse(aBodySlot->mID))
            continue;
        Reanimation *aBody = &aBodySlot->mItem;
        if (aBody->mIsAttachment)
            continue;

        const char *aBlink = nullptr;
        if (aBody->TrackExists("blink_twice") != 0)
            aBlink = "blink_twice";
        else if (aBody->TrackExists("anim_blink") != 0)
            aBlink = "anim_blink";
        else
            continue;

        Reanimation *aBlinkAnim = aHolder->AllocReanimation(0.0f, 0.0f, 0, REANIM_LOADBAR_SPROUT);
        aBlinkAnim->SetAnimRate(4.0f);
        aBlinkAnim->SetFramesForLayer(aBlink);

        if (aBody->TrackExists("anim_face") != 0)
            aBlinkAnim->AttachToAnotherReanimation(aBody, "anim_face");
        else if (aBody->TrackExists("anim_idle") != 0)
            aBlinkAnim->AttachToAnotherReanimation(aBody, "anim_idle");
        else
        {
            aBlinkAnim->ReanimationDelete();
            aResult = 0;
        }
    }
    return aResult;
}

// ==========================================================================
// Board::FadeOutParticles
// @001F8714
// ==========================================================================
void Sexy::Board::FadeOutParticles()
{
    TodParticleHolder *aHolder = mEffectSystem->mParticleHolder;
    for (unsigned int i = 0; i < aHolder->mParticleSystems.mMaxUsedCount; ++i)
    {
        ParticleSystemSlot *aSlot = &ParticleSlots(aHolder)[i];
        if (!SlotInUse(aSlot->mID))
            continue;
        aSlot->mItem.CrossFade("FadeOut");
    }
}

// ==========================================================================
// Board::GetBuddyReanimation
// @001F8850
// ==========================================================================
Reanimation *Sexy::Board::GetBuddyReanimation()
{
    ReanimationHolder *aHolder = mEffectSystem->mReanimationHolder;
    for (unsigned int i = 0; i < aHolder->mMaxUsedCount; ++i)
    {
        ReanimationSlot *aSlot = &ReanimSlots(aHolder)[i];
        if (!SlotInUse(aSlot->mID))
            continue;
        if (!aSlot->mItem.mIsAttachment)
            return &aSlot->mItem;
    }
    return nullptr;
}

// ==========================================================================
// Board::MakeAllBuddyImages
// @001F8890  empty (`bx lr`).
// ==========================================================================
void Sexy::Board::MakeAllBuddyImages() {}

// ==========================================================================
// Board::NewEffectDialog
// @001F8894  empty (`bx lr`).
// ==========================================================================
void Sexy::Board::NewEffectDialog() {}

// ==========================================================================
// Board::ButtonDepress
// @001F8898
// ==========================================================================
void Sexy::Board::ButtonDepress(int theId)
{
    if (theId == 0)
        NewEffectDialog();
}

// ==========================================================================
// Board::RunATest
// @001F88A8
//
// Step the board until a second has passed, then report steps per second.
// ==========================================================================
double Sexy::Board::RunATest()
{
    int aCount = 0;
    Sexy::PerfTimer aTimer;
    aTimer.Start();

    double aDuration;
    do
    {
        ++aCount;
        Update();
        mApp->DrawDirtyStuff();
        aDuration = (double)aTimer.GetDuration();
    } while (aDuration <= 1000.0);

    return (double)aCount / aDuration * 1000.0;
}

// ==========================================================================
// Board::ScaleEffects
// @001F8918
// ==========================================================================
void Sexy::Board::ScaleEffects(float theScale)
{
    TodParticleHolder *aParticles = mEffectSystem->mParticleHolder;
    for (unsigned int i = 0; i < aParticles->mParticleSystems.mMaxUsedCount; ++i)
    {
        ParticleSystemSlot *aSlot = &ParticleSlots(aParticles)[i];
        if (!SlotInUse(aSlot->mID))
            continue;
        aSlot->mItem.OverrideScale(nullptr, theScale);
    }

    ReanimationHolder *aReanims = mEffectSystem->mReanimationHolder;
    for (unsigned int i = 0; i < aReanims->mMaxUsedCount; ++i)
    {
        ReanimationSlot *aSlot = &ReanimSlots(aReanims)[i];
        if (!SlotInUse(aSlot->mID))
            continue;
        if (!aSlot->mItem.mIsAttachment)
            aSlot->mItem.OverrideScale(theScale, theScale);
    }
}

// ==========================================================================
// Board::DoATestSet
// @001F89C4
// ==========================================================================
int Sexy::Board::DoATestSet(float theScale)
{
    ClearEffects();
    MouseDown(100, 100, 1);
    ScaleEffects(theScale);

    ReanimationHolder *aHolder = mEffectSystem->mReanimationHolder;
    for (unsigned int i = 0; i < aHolder->mMaxUsedCount; ++i)
    {
        ReanimationSlot *aSlot = &ReanimSlots(aHolder)[i];
        if (!SlotInUse(aSlot->mID))
            continue;
        int aImages = 0;
        int aPixels = 0;
        CalcReanimPerfInfo(&aSlot->mItem, &aImages, &aPixels);
    }

    ClearEffects();
    ClearDefs();

    // 200 clicks. x runs 100, 102, 105, ... as `i * 500 / 200`.
    for (int i = 0; i != 200; ++i)
        MouseDown(i * 500 / 200 + 100, RandRangeInt(100, 300), 1);

    ScaleEffects(theScale);
    return ClearEffects();
}

// ==========================================================================
// Board::DoReanimatorTest
// @001F8AA4
// ==========================================================================
int Sexy::Board::DoReanimatorTest()
{
    return DoATestSet(1.0f);
}

// ==========================================================================
// Board::MakeBuddyImageStrip
// @001F8B88
//
// Draw the current root reanimation into a 512 canvas at 32 frame steps, crop
// to the inked rectangle, and write BuddyIcons/<name>/IconStrip_<name>_N.png.
// The binary's name buffer is a zeroed 1024-byte stack slot, so the directory
// is "BuddyIcons/" and the files are "IconStrip__N.png".
// ==========================================================================
void Sexy::Board::MakeBuddyImageStrip()
{
    MemoryImage *aCanvas = MakeBuddyCanvas(kBuddyCanvas, kBuddyCanvas);
    Sexy::Graphics aGraphics(aCanvas);
    aGraphics.SetLinearBlend(true);

    Reanimation *aBuddy = GetBuddyReanimation();
    if (aBuddy != nullptr)
    {
        float aSavedTime = AnimTimeOf(aBuddy);
        float aSavedX = 0.0f;
        float aSavedY = 0.0f;
        aBuddy->GetPosition(aSavedX, aSavedY);

        aBuddy->SetPosition(100.0f, 100.0f);
        aBuddy->OverrideScale(kBuddyDrawScale, kBuddyDrawScale);
        for (int i = 0; i != kBuddyFrames; ++i)
        {
            SetAnimTime(aBuddy, (float)i * kBuddyFrameStep);
            aBuddy->Draw(&aGraphics);
        }
        aBuddy->SetPosition(aSavedX, aSavedY);
        aBuddy->OverrideScale(1.0f, 1.0f);
        SetAnimTime(aBuddy, aSavedTime);

        Sexy::TRect<int> aCrop(0, 0, 0, 0);
        FindBuddyImageSize(aCanvas, &aCrop);
        if (aCrop.mWidth != 0)
        {
            MemoryImage *aCropped = MakeBuddyCanvas(aCrop.mWidth, aCrop.mHeight);

            char aName[1024];
            memset(aName, 0, sizeof(aName));

            Sexy::String aDir;
            StrFormat(&aDir, "BuddyIcons/%s", aName);
            MkDir(&aDir);

            aBuddy->SetPosition(100.0f, 100.0f);
            aBuddy->OverrideScale(kBuddyDrawScale, kBuddyDrawScale);
            for (int aFrame = 0; aFrame != kBuddyFrames;)
            {
                ClearCanvas(aCanvas);
                ClearCanvas(aCropped);
                SetAnimTime(aBuddy, (float)aFrame * kBuddyFrameStep);
                aBuddy->Draw(&aGraphics);

                Sexy::Graphics aCropGraphics(aCropped);
                aCropGraphics.DrawImage(aCanvas, 0, 0, aCrop);

                ++aFrame;
                Sexy::String aPath;
                StrFormat(&aPath, "%s/IconStrip_%s_%d.png", aDir.c_str(), aName, aFrame);

                BuddyPng aPng(aCropped);
                ImageLib::WritePNGImage(aPath, &aPng);
            }
            aBuddy->SetPosition(aSavedX, aSavedY);
            aBuddy->OverrideScale(1.0f, 1.0f);
            SetAnimTime(aBuddy, aSavedTime);
            delete aCropped;
        }
    }
    delete aCanvas;
}

// ==========================================================================
// Board::MakeBuddyImage
// @001F8EE8
//
// One frame of the same crop, written as BuddyIcons/<name>/Icon_<name>_WxH.png.
// The name buffer is the same zeroed 1024-byte slot, so the file is
// "BuddyIcons//Icon__WxH.png".
// ==========================================================================
void Sexy::Board::MakeBuddyImage()
{
    MemoryImage *aCanvas = MakeBuddyCanvas(kBuddyCanvas, kBuddyCanvas);
    Sexy::Graphics aGraphics(aCanvas);
    aGraphics.SetLinearBlend(true);

    Reanimation *aBuddy = GetBuddyReanimation();
    if (aBuddy != nullptr)
    {
        float aSavedTime = AnimTimeOf(aBuddy);
        float aSavedX = 0.0f;
        float aSavedY = 0.0f;
        aBuddy->GetPosition(aSavedX, aSavedY);

        aBuddy->SetPosition(100.0f, 100.0f);
        aBuddy->OverrideScale(kBuddyDrawScale, kBuddyDrawScale);
        SetAnimTime(aBuddy, 0.0f);
        aBuddy->Draw(&aGraphics);
        aBuddy->SetPosition(aSavedX, aSavedY);
        aBuddy->OverrideScale(1.0f, 1.0f);
        SetAnimTime(aBuddy, aSavedTime);

        Sexy::TRect<int> aCrop(0, 0, 0, 0);
        FindBuddyImageSize(aCanvas, &aCrop);
        if (aCrop.mWidth != 0)
        {
            MemoryImage *aCropped = MakeBuddyCanvas(aCrop.mWidth, aCrop.mHeight);
            ClearCanvas(aCropped);

            Sexy::Graphics aCropGraphics(aCropped);
            aCropGraphics.DrawImage(aCanvas, 0, 0, aCrop);

            char aName[1024];
            memset(aName, 0, sizeof(aName));

            Sexy::String aDir;
            StrFormat(&aDir, "BuddyIcons/%s", aName);
            MkDir(&aDir);

            Sexy::String aPath;
            StrFormat(&aPath, "%s/Icon_%s_%dx%d.png", aDir.c_str(), aName, aCropped->mWidth, aCropped->mHeight);

            BuddyPng aPng(aCropped);
            ImageLib::WritePNGImage(aPath, &aPng);
            delete aCropped;
        }
    }
    delete aCanvas;
}
