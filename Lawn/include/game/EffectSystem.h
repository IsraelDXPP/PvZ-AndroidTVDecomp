// EffectSystem -- reconstructed from libGameMain.so.
//
// Plain data class: no virtual functions and no vtable. _recon/vtables.json has no
// entry for EffectSystem, and EffectSystem.cpp calls every method through a direct
// address, so nothing here is virtual.
//
// Object size 0x10. The authoritative header PvZ\TodLib\Effect\EffectSystem.h
// declares exactly four pointers, and EffectSystem.cpp touches +0x04, +0x08 and
// +0x0C (the remaining three members). The transcription wrote 0x20, four words too
// many.
//
// Ownership: each holder is created by the framework and freed by
// EffectSystemDispose / EffectSystemFreeAll; TodTriangleGroup lives in the same
// authoritative header but is a separate type.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Attachment.h"
#include "AttachmentHolder.h"
#include "Reanimation.h"
#include "ReanimationHolder.h"
#include "TodParticleHolder.h"
#include "TodParticleSystem.h"
#include "Trail.h"
#include "TrailHolder.h"

class EffectSystem
{
public:
    TodParticleHolder *mParticleHolder;       // +0x00
    TrailHolder *mTrailHolder;                // +0x04
    ReanimationHolder *mReanimationHolder;    // +0x08
    AttachmentHolder *mAttachmentHolder;      // +0x0C
    // size 0x10

    EffectSystem();
    ~EffectSystem();
    int EffectSystemInitialize();
    void EffectSystemDispose();
    unsigned int Update();
    void EffectSystemFreeAll();
    void ProcessDeleteQueue();
};

// Layout check: four pointers (EffectSystem.h of the authoritative tree agrees).
