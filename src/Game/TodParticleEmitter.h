#pragma once

#include "SyncObject.h"
#include "TodAllocator.h"
#include "CoreTypes.h"
#include <cstring>

class TodEmitterDefinition;
class TodParticleSystem;

// Live emitter inside DataArray<TodParticleEmitter>. The array stride is 192.
// The object itself is 0xBC; the trailing id word belongs to the DataArray slot.
// The constructor at 0x209810 zeros the particle list, registers one sync block
// covering +0x28 for 76 bytes, and memsets that region. The definition and the
// owning system are stored by the caller after construction.
class TodParticleEmitter : public SyncObject
{
public:
    struct ParticleIdList
    {
        void *mHead;
        void *mTail;
        int mSize;
        TodAllocator *mpAllocator;
    };

    // The 76 bytes the constructor registers and then zeros. Field order follows
    // the PC emitter (mSpawnAccum, mSystemCenter, counters, overrides) shifted
    // by the TV sync-block start at +0x28.
    struct SyncState
    {
        float mSpawnAccum;                     // +0x28
        SexyVector2 mSystemCenter;             // +0x2C
        int mParticlesSpawned;                 // +0x34
        int mSystemAge;                        // +0x38
        int mSystemDuration;                   // +0x3C
        float mSystemTimeValue;                // +0x40
        float mSystemLastTimeValue;            // +0x44
        bool mDead;                            // +0x48
        unsigned char mPadDead[3];             // +0x49
        Sexy::Color mColorOverride;            // +0x4C
        bool mExtraAdditiveDrawOverride;       // +0x5C
        unsigned char mPadAdditive[3];         // +0x5D
        float mScaleOverride;                  // +0x60
        Sexy::Image *mImageOverride;           // +0x64
        int mCrossFadeEmitterID;               // +0x68
        int mEmitterCrossFadeCountDown;        // +0x6C
        int mFrameOverride;                    // +0x70
    };

    TodEmitterDefinition *mEmitterDef;         // +0x10
    TodParticleSystem *mParticleSystem;        // +0x14
    ParticleIdList mParticleList;              // +0x18
    SyncState mSyncState;                      // +0x28
    // Track and field interpolators, written by TodEmitterInitialize, not by the
    // constructor. 19 system tracks (76 bytes) then 4 fields times two floats.
    // System-track interpolators. The constructor does not write them.
    // Eighteen floats fill +0x74 through the end of the 0xBC object.
    float mTrackInterp[18];                    // +0x74

    TodParticleEmitter()
    {
        mParticleList.mHead = nullptr;
        mParticleList.mTail = nullptr;
        mParticleList.mSize = 0;
        mParticleList.mpAllocator = nullptr;
        SyncBlockInfo aInfo;
        aInfo.mData = &mSyncState;
        aInfo.mSize = 76;
        mSyncBlocks.push_back(aInfo);
        std::memset(&mSyncState, 0, sizeof(mSyncState));
    }

    // Bodies are still the free-function stubs in ReconVariadic.cpp.
    static int CrossFadeEmitter(...);
    static int DeleteAll(...);
    static int Draw(...);
    static int SystemMove(...);
    static int Update(...);
    static int TodEmitterInitialize(...);
};

struct ParticleEmitterSlot
{
    TodParticleEmitter mItem;
    unsigned int mID;
};
