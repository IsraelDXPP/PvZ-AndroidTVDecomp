// TrashBin -- reconstructed from libGameMain.so.
//
// Inheritance: `class TrashBin : public Sexy::Widget`. The authoritative header
// PvZ\Lawn\Widget\TrashBin.h says so, _ZTV8TrashBin @0x680xxx holds Widget's 122
// slots with only slot 36 replaced, and TrashBin.cpp:63 already does
// `new ((Sexy::Widget *)this) Sexy::Widget();` by hand.
//
// SIZE 0x158 -- the authoritative header closes with "86 ints" = 344 = 0x158, and
// this binary's own member accesses agree field for field (see below). The
// transcriber's `mObject[0x158]` happened to be right; only the names were gone.
//
// ---------------------------------------------------------------------------
// VTABLE @0x680xxx, 122 primary slots, all inherited except:
//   [  0] 0x143cd4  ~TrashBin                override
//   [  1]           (ARM D0; MSVC x86 keeps ONE dtor slot)
//   [ 36] 0x143cd0  Draw(Sexy::Graphics *)   override
// The authoritative header also lists `_constructor`/`_destructor` hooks, which
// are the hook project's wrappers, not additional vtable slots.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Graphics.h"
#include "Image.h"
#include "LawnPlayerInfo.h"
#include "Widget.h"

// The two pile heights, from the authoritative header. They are free constants,
// not members.
inline constexpr int zombiePileHeight = 70;
inline constexpr int plantPileHeight = 100;

class TrashBin : public Sexy::Widget
{
public:
    enum TrashPileType
    {
        PLANT_PILE = 0,
        ZOMBIE_PILE = 1,
    };

    // ---- members, absolute offsets ----
    // The authoritative header's ordinals put mTrashPileType at 0x104; the bodies
    // put it at 0x100 (TrashBin.cpp:63 writes it there, and every later field is
    // 4 bytes lower than the ordinal table predicts), so the byte offsets below
    // follow the bodies.
    TrashPileType mTrashPileType; // +0x100
    float mTrashHeight;           // +0x104
    Sexy::Image * mPileImage[10]; // +0x108 .. +0x12F
    bool mMirrorPile[10];         // +0x130 .. +0x139
    int mRandomPileWeight[6];     // +0x13C .. +0x153
    int mPileNum;                 // +0x154
    // size 0x158

    virtual ~TrashBin();                                  // [  0]
    virtual void Draw(Sexy::Graphics * g);                // [ 36]

    TrashBin(TrashPileType theTrashPileType, float theHeight);
    Sexy::Image * GetZombieTrashPiece(int theLevel);
    Sexy::Image * GetPlantTrashPiece(int theLevel);
};

