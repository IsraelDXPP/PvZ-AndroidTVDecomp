#pragma once

#include "ReconTypes.h"
#include "Image.h"

enum ParticleEmitterType
{
    EMITTER_CIRCLE,
    EMITTER_BOX,
    EMITTER_BOX_PATH,
    EMITTER_CIRCLE_PATH,
    EMITTER_CIRCLE_EVEN_SPACING
};

enum ParticleFieldType
{
    FIELD_INVALID,
    FIELD_FRICTION,
    FIELD_ACCELERATION,
    FIELD_ATTRACTOR,
    FIELD_MAX_VELOCITY,
    FIELD_VELOCITY,
    FIELD_POSITION,
    FIELD_SYSTEM_POSITION,
    FIELD_GROUND_CONSTRAINT,
    FIELD_SHAKE,
    FIELD_CIRCLE,
    FIELD_AWAY
};

enum ParticleFlags
{
    PARTICLE_RANDOM_LAUNCH_SPIN = 1,
    PARTICLE_ALIGN_LAUNCH_SPIN = 2,
    PARTICLE_PIXEL_ALIGNED = 4,
    PARTICLE_SYSTEM_LOOPS = 8,
    PARTICLE_PARTICLE_LOOPS = 16,
    PARTICLE_PARTICLES_DONT_FOLLOW = 32,
    PARTICLE_RANDOM_START_TIME = 64,
    PARTICLE_DIE_IF_OVERLOADED = 128,
    PARTICLE_ADDITIVE = 256,
    PARTICLE_FULL_SCREEN = 512,
    PARTICLE_SOFTWARE_ONLY = 1024,
    PARTICLE_HARDWARE_ONLY = 2048
};

struct ParticleField
{
    ParticleFieldType mFieldType;
    FloatParameterTrack mX;
    FloatParameterTrack mY;
};

// Definition cache layout and tracks verified against 0x207B4C / 0x209BF0.
class TodEmitterDefinition
{
public:
    Sexy::Image* mImage;
    int mImageCol;
    int mImageRow;
    int mImageFrames;
    int mAnimated;
    int mFlags;
    ParticleEmitterType mEmitterType;
    const char* mName;
    const char* mOnDuration;
    FloatParameterTrack mSystemDuration;
    FloatParameterTrack mCrossFadeDuration;
    FloatParameterTrack mSpawnRate;
    FloatParameterTrack mSpawnMinActive;
    FloatParameterTrack mSpawnMaxActive;
    FloatParameterTrack mSpawnMaxLaunched;
    FloatParameterTrack mEmitterRadius;
    FloatParameterTrack mEmitterOffsetX;
    FloatParameterTrack mEmitterOffsetY;
    FloatParameterTrack mEmitterBoxX;
    FloatParameterTrack mEmitterBoxY;
    FloatParameterTrack mEmitterSkewX;
    FloatParameterTrack mEmitterSkewY;
    FloatParameterTrack mEmitterPath;
    FloatParameterTrack mParticleDuration;
    FloatParameterTrack mLaunchSpeed;
    FloatParameterTrack mLaunchAngle;
    FloatParameterTrack mSystemRed;
    FloatParameterTrack mSystemGreen;
    FloatParameterTrack mSystemBlue;
    FloatParameterTrack mSystemAlpha;
    FloatParameterTrack mSystemBrightness;
    ParticleField* mParticleFields;
    int mParticleFieldCount;
    ParticleField* mSystemFields;
    int mSystemFieldCount;
    FloatParameterTrack mParticleRed;
    FloatParameterTrack mParticleGreen;
    FloatParameterTrack mParticleBlue;
    FloatParameterTrack mParticleAlpha;
    FloatParameterTrack mParticleBrightness;
    FloatParameterTrack mParticleSpinAngle;
    FloatParameterTrack mParticleSpinSpeed;
    FloatParameterTrack mParticleScale;
    FloatParameterTrack mParticleStretch;
    FloatParameterTrack mCollisionReflect;
    FloatParameterTrack mCollisionSpin;
    FloatParameterTrack mClipTop;
    FloatParameterTrack mClipBottom;
    FloatParameterTrack mClipLeft;
    FloatParameterTrack mClipRight;
    FloatParameterTrack mAnimationRate;
};

class TodParticleDefinition
{
public:
    TodEmitterDefinition* mEmitterDefs;
    int mEmitterDefCount;

    TodParticleDefinition();
};
