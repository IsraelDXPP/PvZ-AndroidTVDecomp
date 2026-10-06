#pragma once

#include "SyncObject.h"
#include "DataArray.h"
#include "TodAllocator.h"
#include "TodParticle.h"
#include "TodParticleDefinition.h"

class TodParticleSystem;

enum ParticleSystemTracks
{
    SYSTEM_TRACK_SPAWN_RATE,
    SYSTEM_TRACK_SPAWN_MIN_ACTIVE,
    SYSTEM_TRACK_SPAWN_MAX_ACTIVE,
    SYSTEM_TRACK_SPAWN_MAX_LAUNCHED,
    SYSTEM_TRACK_EMITTER_PATH,
    SYSTEM_TRACK_RED,
    SYSTEM_TRACK_GREEN,
    SYSTEM_TRACK_BLUE,
    SYSTEM_TRACK_ALPHA,
    SYSTEM_TRACK_BRIGHTNESS,
    SYSTEM_TRACK_COUNT
};

struct ParticleRenderParams
{
    bool mRedSet;
    bool mGreenSet;
    bool mBlueSet;
    bool mAlphaSet;
    bool mScaleSet;
    bool mStretchSet;
    bool mSpinSet;
    bool mPositionSet;
    float mRed;
    float mGreen;
    float mBlue;
    float mAlpha;
    float mScale;
    float mStretch;
    float mSpin;
    float mPositionX;
    float mPositionY;
};

// Runtime state verified against 0x209810 and 0x20A42C.
class TodParticleEmitter : public SyncObject
{
public:
    struct ParticleIdList
    {
        DataArrayIdNode* mHead;
        DataArrayIdNode* mTail;
        int mSize;
        TodAllocator* mpAllocator;
    };

    struct SyncState
    {
        float mSpawnAccum;
        SexyVector2 mSystemCenter;
        int mParticlesSpawned;
        int mSystemAge;
        int mSystemDuration;
        float mSystemTimeValue;
        float mSystemLastTimeValue;
        bool mDead;
        Sexy::Color mColorOverride;
        bool mExtraAdditiveDrawOverride;
        float mScaleOverride;
        Sexy::Image* mImageOverride;
        unsigned int mCrossFadeEmitterID;
        int mEmitterCrossFadeCountDown;
        int mFrameOverride;
    };

    TodEmitterDefinition* mEmitterDef;
    TodParticleSystem* mParticleSystem;
    ParticleIdList mParticleList;
    SyncState mSyncState;
    float mTrackInterp[SYSTEM_TRACK_COUNT];
    float mSystemFieldInterp[4][2];

    TodParticleEmitter();
    void CrossFadeEmitter(TodParticleEmitter* theEmitter);
    void DeleteAll();
    void Draw(Sexy::Graphics* theGraphics);
    void SystemMove(float theX, float theY);
    void Update();
    void TodEmitterInitialize(float theX, float theY, TodParticleSystem* theSystem,
                              TodEmitterDefinition* theDefinition);
    TodParticle* SpawnParticle(int theIndex, int theCount);
    bool UpdateParticle(TodParticle* theParticle);
    void DeleteParticle(TodParticle* theParticle);
    void UpdateSpawning();
    bool CrossFadeParticle(TodParticle* theParticle, TodParticleEmitter* theEmitter);
    bool CrossFadeParticleToName(TodParticle* theParticle, const char* theName);
    void UpdateParticleField(TodParticle* theParticle, ParticleField& theField, int theIndex);
    void UpdateSystemField(ParticleField& theField, int theIndex);
    float SystemTrackEvaluate(FloatParameterTrack& theTrack, ParticleSystemTracks theIndex);
    float ParticleTrackEvaluate(FloatParameterTrack& theTrack, TodParticle* theParticle, ParticleTracks theIndex);
    void GetRenderParams(TodParticle* theParticle, ParticleRenderParams& theParams);
    void DrawParticle(Sexy::Graphics* theGraphics, TodParticle* theParticle);
};

struct ParticleEmitterSlot
{
    TodParticleEmitter mItem;
    unsigned int mID;
};
