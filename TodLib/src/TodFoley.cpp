// TodFoley -- the foley (sound-effect) system.
//
// The 110-record store itself is Game\TodFoley.h; this file holds the bodies.
// Every stride below is the header's own (a FoleyTypeData record is 164 bytes,
// a FoleyTypeDataElement 20) and the field names come from
// Game\FoleyTypeData.h. The sound instance is Sexy::SoundInstance. Calls go through its virtual
// methods, so the Windows override (SDLSoundInstance) is the one that runs.
// The app sound device is Sexy::SoundManager, already a virtual interface.

#include "TodFoley.h"

#include "ReconTypes.h"
#include "ReconData.h"
#include "ReconFreeFunctions.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"
#include "SoundManager.h"

namespace
{

inline Sexy::SexyAppBase *GetApp()
{
    return (Sexy::SexyAppBase *)gSexyAppBase;
}

inline Sexy::SoundManager *GetSoundManager()
{
    Sexy::SexyAppBase *anApp = GetApp();
    if (anApp == nullptr)
        return nullptr;
    return anApp->mSoundManager;
}

} // namespace

// ==========================================================================
// FoleyParams *LookupFoley(FoleyType)
// @001FF778  4 bytes
//
// `&gFoleyParamArray[theFoleyType]` -- the one-line table index the whole file
// goes through. Declared by ReconFreeFunctions.h; the variadic
// `int LookupFoley(...)` stub there stays untouched because this fixed-parameter
// overload is the better match.
// ==========================================================================
FoleyParams *LookupFoley(FoleyType theFoleyType)
{
    return &gFoleyParamArray[theFoleyType];
}

// ==========================================================================
// void SoundSystemReleaseFinishedInstances(TodFoley *)
// @001FF794  124 bytes
//
// One pass over every instance of every foley type: an instance whose reference
// count has dropped to zero, that is not parked by a pause, and whose sound has
// stopped playing is released and its slot cleared.
// ==========================================================================
void SoundSystemReleaseFinishedInstances(TodFoley *theSoundSystem)
{
    for (int aFoleyIndex = 0; aFoleyIndex < (int)gFoleyParamArraySize; ++aFoleyIndex)
    {
        FoleyTypeData &aTypeData = theSoundSystem->mTypeData[aFoleyIndex];

        for (int anInstanceIndex = 0; anInstanceIndex < FoleyTypeData::MAX_FOLEY_INSTANCES;
             ++anInstanceIndex)
        {
            FoleyTypeDataElement &anInstance = aTypeData.mFoleyInstances[anInstanceIndex];

            if (anInstance.mRefCount == 0)
                continue;
            if (anInstance.mPaused)
                continue;
            if (anInstance.mInstance->IsPlaying())
                continue;

            anInstance.mInstance->Release();
            anInstance.mInstance = nullptr;
            anInstance.mRefCount = 0;
        }
    }
}

// ==========================================================================
// FoleyTypeDataElement *SoundSystemFindInstance(TodFoley *, FoleyType)
// @001FF810  26 bytes
//
// The first instance of this foley type with a live reference count, or null.
// ==========================================================================
FoleyTypeDataElement *SoundSystemFindInstance(TodFoley *theSoundSystem, FoleyType theFoleyType)
{
    FoleyTypeDataElement *anInstances = theSoundSystem->mTypeData[theFoleyType].mFoleyInstances;

    for (int anInstanceIndex = 0; anInstanceIndex < FoleyTypeData::MAX_FOLEY_INSTANCES;
         ++anInstanceIndex)
    {
        if (anInstances[anInstanceIndex].mRefCount > 0)
            return &anInstances[anInstanceIndex];
    }
    return nullptr;
}

// ==========================================================================
// bool SoundSystemHasFoleyPlayedTooRecently(TodFoley *, FoleyType)
// @001FF82C  32 bytes
//
// True while any instance of this foley type was started within the last ten
// frames, which is what stops a repeated effect from stacking up on itself.
// ==========================================================================
bool SoundSystemHasFoleyPlayedTooRecently(TodFoley *theSoundSystem, FoleyType theFoleyType)
{
    FoleyTypeDataElement *anInstances = theSoundSystem->mTypeData[theFoleyType].mFoleyInstances;

    for (int anInstanceIndex = 0; anInstanceIndex < FoleyTypeData::MAX_FOLEY_INSTANCES;
         ++anInstanceIndex)
    {
        if (anInstances[anInstanceIndex].mRefCount != 0 &&
            GetApp()->GetFrameCounter() - anInstances[anInstanceIndex].mStartTime <= 9)
            return true;
    }
    return false;
}

// ==========================================================================
// FoleyTypeDataElement *SoundSystemGetFreeInstanceIndex(TodFoley *, FoleyType)
// @001FF86C  24 bytes
//
// The first instance of this foley type with a free slot, or null when all
// eight are taken.
// ==========================================================================
FoleyTypeDataElement *SoundSystemGetFreeInstanceIndex(TodFoley *theSoundSystem,
                                                      FoleyType theFoleyType)
{
    FoleyTypeDataElement *anInstances = theSoundSystem->mTypeData[theFoleyType].mFoleyInstances;

    for (int anInstanceIndex = 0; anInstanceIndex < FoleyTypeData::MAX_FOLEY_INSTANCES;
         ++anInstanceIndex)
    {
        if (anInstances[anInstanceIndex].mRefCount == 0)
            return &anInstances[anInstanceIndex];
    }
    return nullptr;
}

// ==========================================================================
// TodFoley::StopFoley(FoleyType)
// @001FF884  42 bytes
// ==========================================================================
FoleyTypeDataElement *TodFoley::StopFoley(FoleyType theFoleyType)
{
    SoundSystemReleaseFinishedInstances(this);

    FoleyTypeDataElement *anInstance = SoundSystemFindInstance(this, theFoleyType);
    if (anInstance == nullptr)
        return nullptr;

    // The last reference going away releases the sound itself.
    if (--anInstance->mRefCount == 0)
    {
        anInstance->mInstance->Release();
        anInstance->mInstance = nullptr;
    }
    return anInstance;
}

// ==========================================================================
// TodFoley::GamePause(bool)
// @001FF8B0  156 bytes
//
// The pause flag lives on each instance, so pausing walks the whole store:
// every live instance of every type whose FoleyParams carry MF_LOOP (bit 2 of
// mFoleyFlags) is stopped and parked, and un-pausing restarts it.
// ==========================================================================
void TodFoley::GamePause(bool thePause)
{
    SoundSystemReleaseFinishedInstances(this);

    for (int aFoleyIndex = 0; aFoleyIndex < (int)gFoleyParamArraySize; ++aFoleyIndex)
    {
        const FoleyParams *aFoleyParams = LookupFoley((FoleyType)aFoleyIndex);
        if ((aFoleyParams->mFoleyFlags & 4) == 0)
            continue;

        FoleyTypeData &aTypeData = mTypeData[aFoleyIndex];

        for (int anInstanceIndex = 0; anInstanceIndex < FoleyTypeData::MAX_FOLEY_INSTANCES;
             ++anInstanceIndex)
        {
            FoleyTypeDataElement &anInstance = aTypeData.mFoleyInstances[anInstanceIndex];

            if (anInstance.mRefCount == 0)
                continue;

            // TV restarts only parked sounds when resuming. Pausing always
            // stops the active instance and clears its saved offset.
            if (!thePause && !anInstance.mPaused)
                continue;

            Sexy::SoundInstance *aSound = anInstance.mInstance;
            anInstance.mPaused = thePause;

            if (thePause)
            {
                // Parking: the instance is stopped and its pause offset cleared,
                // so un-pausing restarts it from the beginning.
                anInstance.mPauseOffset = 0;
                aSound->Stop();
            }
            else
            {
                aSound->Play((aFoleyParams->mFoleyFlags & 1) != 0, false);
            }
        }
    }
}

// ==========================================================================
// TodFoley::CancelPausedFoley()
// @001FF954  106 bytes
// ==========================================================================
void TodFoley::CancelPausedFoley()
{
    SoundSystemReleaseFinishedInstances(this);

    for (int aFoleyIndex = 0; aFoleyIndex < (int)gFoleyParamArraySize; ++aFoleyIndex)
    {
        FoleyTypeData &aTypeData = mTypeData[aFoleyIndex];

        for (int anInstanceIndex = 0; anInstanceIndex < FoleyTypeData::MAX_FOLEY_INSTANCES;
             ++anInstanceIndex)
        {
            FoleyTypeDataElement &anInstance = aTypeData.mFoleyInstances[anInstanceIndex];

            if (anInstance.mRefCount == 0 || !anInstance.mPaused)
                continue;

            // Drop the pause and release the sound outright: this is the path
            // that abandons a parked effect rather than resuming it.
            Sexy::SoundInstance *aSound = anInstance.mInstance;
            anInstance.mRefCount = 0;
            aSound->Release();
            anInstance.mInstance = nullptr;
        }
    }
}

// ==========================================================================
// TodFoley::ApplyMusicVolume(FoleyTypeDataElement *)
// @001FF9C8  64 bytes
//
// The music volume is the app's music volume scaled by the ratio of its music
// volume to its SFX volume (+0x58 and +0x60). Below an epsilon of 1e-6 the ratio
// is treated as zero and the sound is silenced outright; the binary also reads
// the vptr into r1 and compares it against itself, which is the null check the
// compiler emitted for the virtual call.
// ==========================================================================
void TodFoley::ApplyMusicVolume(FoleyTypeDataElement *theInstance)
{
    Sexy::SoundInstance *aSound = theInstance->mInstance;

    if (GetApp()->mSfxVolume < 0.000000999999997)
        aSound->SetVolume(0.0);
    else
        aSound->SetVolume(GetApp()->mMusicVolume / GetApp()->mSfxVolume);
}

// ==========================================================================
// TodFoley::PlayFoleyPitch(FoleyType,float)
// @001FFA18  308 bytes
//
// Pick a variation, load its sound, start it, and return the instance. The
// variation choice honours MF_RANDOM_START (bit 4 of mFoleyFlags): with it set,
// the variation last used for this type is excluded from the pick, so the same
// variation never plays twice in a row.
// ==========================================================================
FoleyTypeDataElement *TodFoley::PlayFoleyPitch(FoleyType theFoleyType, float thePitch)
{
    const FoleyParams *aFoleyParams = LookupFoley(theFoleyType);
    SoundSystemReleaseFinishedInstances(this);

    if (SoundSystemHasFoleyPlayedTooRecently(this, theFoleyType) && (aFoleyParams->mFoleyFlags & 1) == 0)
        return nullptr;

    if ((aFoleyParams->mFoleyFlags & 2) != 0)
    {
        // An instance of this effect is already running and the params ask for
        // it to be shared rather than restarted: take another reference and
        // re-stamp it.
        FoleyTypeDataElement *aRunning = SoundSystemFindInstance(this, theFoleyType);
        if (aRunning != nullptr)
        {
            ++aRunning->mRefCount;
            aRunning->mStartTime = GetApp()->GetFrameCounter();
            return aRunning;
        }
    }

    FoleyTypeDataElement *anInstance = SoundSystemGetFreeInstanceIndex(this, theFoleyType);
    if (anInstance == nullptr)
        return nullptr;

    // Collect the variations other than the last one used, then pick one.
    int aVariations[10];
    int aVariationCount = 0;

    for (int aVariation = 0; aVariation < 10; ++aVariation)
    {
        if ((aFoleyParams->mFoleyFlags & 0x10) != 0 &&
            mTypeData[theFoleyType].mLastVariationPlayed == aVariation)
            continue;
        if (aFoleyParams->mSfxID[aVariation] == nullptr)
            break;

        aVariations[aVariationCount++] = aVariation;
    }

    if (aVariationCount == 0)
        return nullptr;

    Sexy::SoundManager *aSoundManager = GetSoundManager();
    if (aSoundManager == nullptr)
        return nullptr;

    const int aVariation = TodPickFromArray(aVariations, aVariationCount);
    const int aSoundId = *aFoleyParams->mSfxID[aVariation];
    if (aSoundId < 0)
        return nullptr;

    Sexy::SoundInstance *aSound =
        aSoundManager->GetSoundInstance(aSoundId);
    if (aSound == nullptr)
        return nullptr;

    anInstance->mInstance = aSound;
    anInstance->mRefCount = 1;
    anInstance->mStartTime = GetApp()->GetFrameCounter();
    mTypeData[theFoleyType].mLastVariationPlayed = aVariation;

    if (thePitch != 0.0f)
        aSound->AdjustPitch(thePitch);

    if ((aFoleyParams->mFoleyFlags & 8) != 0)
    {
        ApplyMusicVolume(anInstance);
        // ApplyMusicVolume can release the instance it was handed, so the
        // params are re-read before the flags are used below.
        aFoleyParams = LookupFoley(theFoleyType);
    }

    aSound->Play((aFoleyParams->mFoleyFlags & 1) != 0, false);
    return anInstance;
}

// ==========================================================================
// TodFoley::PlayFoley(FoleyType)
// @001FFB54  52 bytes
// ==========================================================================
FoleyTypeDataElement *TodFoley::PlayFoley(FoleyType theFoleyType)
{
    const FoleyParams *aFoleyParams = LookupFoley(theFoleyType);

    // `Sexy::Rand(float)` @0x2F9EE4, which is what the dump's stray second
    // argument to Rand stands for. A zero pitch range means no randomisation.
    const float aPitch =
        (aFoleyParams->mPitchRange == 0.0f) ? 0.0f : Rand(aFoleyParams->mPitchRange);

    return PlayFoleyPitch(theFoleyType, aPitch);
}

// ==========================================================================
// TodFoley::RehookupSoundWithMusicVolume()
// @001FFB88  94 bytes
//
// Re-apply the music volume to every live instance of the effects that the
// params mark as music-scaled (bit 3 of mFoleyFlags), which is how a volume
// change reaches sounds that are already playing.
// ==========================================================================
int TodFoley::RehookupSoundWithMusicVolume()
{
    SoundSystemReleaseFinishedInstances(this);

    for (int aFoleyIndex = 0; aFoleyIndex < (int)gFoleyParamArraySize; ++aFoleyIndex)
    {
        const FoleyParams *aFoleyParams = LookupFoley((FoleyType)aFoleyIndex);
        if ((aFoleyParams->mFoleyFlags & 8) == 0)
            continue;

        FoleyTypeData &aTypeData = mTypeData[aFoleyIndex];

        for (int anInstanceIndex = 0; anInstanceIndex < FoleyTypeData::MAX_FOLEY_INSTANCES;
             ++anInstanceIndex)
        {
            FoleyTypeDataElement &anInstance = aTypeData.mFoleyInstances[anInstanceIndex];

            if (anInstance.mRefCount != 0)
                ApplyMusicVolume(&anInstance);
        }
    }
    return 0;
}

// ==========================================================================
// TodFoley::IsFoleyPlaying(FoleyType)
// @001FFBF0  26 bytes
// ==========================================================================
bool TodFoley::IsFoleyPlaying(FoleyType theFoleyType)
{
    SoundSystemReleaseFinishedInstances(this);
    return SoundSystemFindInstance(this, theFoleyType) != nullptr;
}
