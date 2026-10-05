#pragma once

#include "ReconTypes.h"
#include "TodAllocator.h"
#include "DataArray.h"

class TodParticleSystem;
class TodParticleEmitter;
class TodParticle;

// Three DataArray headers and two node pools. EffectSystemInitialize allocates
// the holder with operator new(0x7C). The arrays are named "particle systems",
// "emitters" and "particles". The pools hand out 12-byte list nodes: the first
// pool is the particle lists, the second is the emitter lists.
class TodParticleHolder
{
public:
    DataArray<TodParticleSystem, ParticleSystemSlot> mParticleSystems;
    DataArray<TodParticleEmitter, ParticleEmitterSlot> mEmitters;
    DataArray<TodParticle, ParticleSlot> mParticles;
    TodAllocator mParticleNodes;
    TodAllocator mEmitterNodes;

    TodParticleHolder();
    void InitializeHolder();
    bool IsOverLoaded();
    void DisposeHolder();
    ~TodParticleHolder();

    TodParticleSystem *AllocParticleSystemFromDef(
        float theX, float theY, int theRenderOrder,
        TodParticleDefinition *theDefinition, ParticleEffect theParticleEffect);
    TodParticleSystem *AllocParticleSystem(
        float theX, float theY, int theRenderOrder, ParticleEffect theParticleEffect);
};
