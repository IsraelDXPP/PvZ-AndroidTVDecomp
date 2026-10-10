#ifndef LAWNPROJECTTV_TODLIB_INCLUDE_TODPARTICLE_H
#define LAWNPROJECTTV_TODLIB_INCLUDE_TODPARTICLE_H

#include "SyncObject.h"

class TodParticleEmitter;

enum ParticleTracks
{
    PARTICLE_TRACK_RED,
    PARTICLE_TRACK_GREEN,
    PARTICLE_TRACK_BLUE,
    PARTICLE_TRACK_ALPHA,
    PARTICLE_TRACK_BRIGHTNESS,
    PARTICLE_TRACK_SPIN_SPEED,
    PARTICLE_TRACK_SPIN_ANGLE,
    PARTICLE_TRACK_SCALE,
    PARTICLE_TRACK_STRETCH,
    PARTICLE_TRACK_COLLISION_REFLECT,
    PARTICLE_TRACK_COLLISION_SPIN,
    PARTICLE_TRACK_CLIP_TOP,
    PARTICLE_TRACK_CLIP_BOTTOM,
    PARTICLE_TRACK_CLIP_LEFT,
    PARTICLE_TRACK_CLIP_RIGHT,
    PARTICLE_TRACK_ANIMATION_RATE,
    PARTICLE_TRACK_COUNT
};

class TodParticle : public SyncObject
{
public:
    struct SyncState
    {
        int mParticleDuration;
        int mParticleAge;
        float mParticleTimeValue;
        float mParticleLastTimeValue;
        float mAnimationTimeValue;
        SexyVector2 mVelocity;
        SexyVector2 mPosition;
        int mImageFrame;
        float mSpinPosition;
        float mSpinVelocity;
        unsigned int mCrossFadeParticleID;
        int mCrossFadeDuration;
        float mTrackInterp[PARTICLE_TRACK_COUNT];
        float mFieldInterp[4][2];
    };

    TodParticleEmitter *mParticleEmitter;
    SyncState mSyncState;

    TodParticle();
    virtual ~TodParticle();
};

struct ParticleSlot
{
    TodParticle mItem;
    unsigned int mID;
};

#endif // LAWNPROJECTTV_TODLIB_INCLUDE_TODPARTICLE_H
