#pragma once

#include "ReconTypes.h"
#include "SyncObject.h"
#include "TodParticleHolder.h"

class TodParticleDefinition;
class TodEmitterDefinition;
class TodAllocator;
class TodParticleEmitter;

// SyncObject plus the derived members. The DataArray stride is 0x3C because the
// slot stores an id word after the 0x38-byte object. Update, Draw, SystemMove
// and the Override family are not virtual; only the destructor is.
class TodParticleSystem : public SyncObject
{
public:
    virtual ~TodParticleSystem();

    struct EmitterIdList
    {
        DataArrayIdNode* mHead;
        DataArrayIdNode* mTail;
        int mSize;
        TodAllocator *mpAllocator;
    };

    ParticleEffect mEffectType;
    TodParticleDefinition *mParticleDef;
    TodParticleHolder *mParticleHolder;
    EmitterIdList mEmitterList;
    bool mDead;
    bool mIsAttachment;
    int mRenderOrder;
    bool mDontUpdate;
    bool mActive;

    void ParticleSystemDie();
    void Draw(Sexy::Graphics *theGraphics);
    void SystemMove(float theX, float theY);
    void OverrideColor(const char *theEmitterName, const Sexy::Color &theColor);
    void OverrideExtraAdditiveDraw(const char *theEmitterName, bool theEnableExtraAdditiveDraw);
    void OverrideImage(const char *theEmitterName, Sexy::Image *theImage);
    void OverrideFrame(const char *theEmitterName, int theFrame);
    void OverrideScale(const char *theEmitterName, float theScale);
    TodParticleEmitter *FindEmitterByName(const char *theEmitterName);
    TodEmitterDefinition *FindEmitterDefByName(const char *theEmitterName);
    TodParticleSystem();
    void TodParticleInitializeFromDef(
        float theX, float theY, int theRenderOrder,
        TodParticleDefinition *theDefinition, ParticleEffect theEffectType);
    void Update();
    void CrossFade(const char *theEmitterName);
};

struct ParticleSystemSlot
{
    TodParticleSystem mItem;
    unsigned int mID;
};
