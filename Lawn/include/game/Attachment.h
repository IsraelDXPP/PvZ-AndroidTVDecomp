// Attachment -- reconstructed from libGameMain.so.
//
// No base class. The authoritative header PvZ\TodLib\Effect\Attachment.h declares
// no base either, but the class IS polymorphic: the constructor @0x1F5D64 stores
// the vtable at +0x00 (`*(_DWORD *)this = &off_686990;`) and the destructor
// @0x1F5718 puts it back, so the Itanium object has a four-byte vptr at +0x00 and
// the authoritative header's `int unk[4]` prefix covers that vptr plus three
// unknown words. The destructor also has a deleting form (_ZN10AttachmentD0Ev),
// which only exists for a virtual destructor.
//
// SIZE 0x31C -- the authoritative header closes with "199 ints" = 796 = 0x31C,
// and the bodies use `this + 796` as the end of the object. On MSVC x86 the
// virtual destructor fills the same leading slot, so the explicit member table
// carries the three remaining prefix words:
//     4 (vptr) + 12 (unk[3]) + 16 * 0x30 (mEffectArray) + 4 + 1 + 1 + 1 + 1 pad + 4
//     = 0x31C
//
// ---------------------------------------------------------------------------
// VTABLE, two ARM slots for the destructor only:
//   [  0] 0x1f5660  ~Attachment
//   [  1] 0x1f56a8  ~Attachment (ARM D0; MSVC x86 keeps ONE dtor slot)

#pragma once

#include <vector>

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Graphics.h"
#include "Reanimation.h"
#include "SexyTransform2D.h"
#include "TodParticleSystem.h"
#include "Trail.h"

// One attached effect. 0x30 bytes because SexyTransform2D is nine floats (0x24)
// and the two trailing bools pad the record to its alignment.
// TODO(recon): the authoritative header types mEffectType as `EffectType`, an
// enum Sexy\ReconTypes.h does not define yet; the bodies only compare it against
// plain integers.
class AttachEffect
{
public:
    ReanimationID mEffectID;       // +0x00
    int mEffectType;               // +0x04
    Sexy::SexyTransform2D mOffset; // +0x08 .. +0x2B
    bool mDontDrawIfParentHidden;  // +0x2C
    bool mDontPropogateColor;      // +0x2D
};

class Attachment
{
public:
    // +0x00 is the vtable pointer the constructor stores and the destructor
    // restores (see the header comment); the authoritative header's `int unk[4]`
    // prefix therefore contributes only three explicit words under MSVC.
    //
    // Those three words are not unknown: they are the `std::vector<SyncBlockInfo>`
    // the constructor registers the effect array with. `Attachment::Attachment`
    // @0x1F5D04 builds a SyncBlockInfo over the effect array (data pointer, then
    // the size 773) and calls `_M_insert_aux` on the vector at +0x04, exactly as
    // every GameObject-derived class registers its own sync block. The three
    // words are libstdcxx::vector's begin/end/capacity triple, so naming them
    // costs no size: 3 * 4 == 12 either way.
    std::vector<SyncBlockInfo> mSyncBlocks;
    AttachEffect mEffectArray[16]; // +0x10 .. +0x30F
    int mNumEffects;               // +0x310
    bool mDead;                    // +0x314
    bool mActive;                  // +0x315
    bool mUsesClipping;            // +0x316
    int mAttachmentID;             // +0x318
    // size 0x31C

    virtual ~Attachment();         // [ 0]

    // Return types come from the ARM, not from the dump's scratch register:
    // Update @0x1f4b1c, SetPosition @0x1f4c80, OverrideColor @0x1f4dd8,
    // CrossFade @0x1f50f8, SetMatrix @0x1f515c, Draw @0x1f52d8, Detach @0x1f541c
    // and AttachmentDie @0x1f5524 all end `POP {..,PC}` with r0 never set, so they
    // are void. PropogateColor @0x1f4ec8 does set r0 (the dump's `result` word) and
    // stays `unsigned int`. None of these is virtual, so the types do not affect
    // the object's layout or its two-slot vtable.
    void Update();
    void SetPosition(const Sexy::SexyVector2 &thePosition);
    void OverrideColor(const Color & theColor);
    unsigned int PropogateColor(const Color &theColor, bool theEnableAdditive, const Color &theAdditiveColor, bool theEnableOverlay, const Color &theOverlayColor);
    void OverrideScale(float theScale);
    void CrossFade(const char * theCrossFadeName);
    void SetMatrix(const Sexy::SexyTransform2D & theMatrix);
    void Draw(Sexy::Graphics * g, bool theParentHidden);
    void Detach();
    void AttachmentDie();
    Attachment();
};

// Layout check: 4 + 16 effect records + the tail.
