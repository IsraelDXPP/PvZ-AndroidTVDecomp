// CursorObject -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class CursorObject : public GameObject` (GameObject : SyncObject).
// The base occupies +0x00..+0x33, so the derived members start at +0x34.
// Total size 0x5C, from the allocation site `operator new(0x5Cu)` at Board.cpp:18124;
// the authoritative header PvZ\Lawn\Board\CursorObject.h agrees ("size = 23 ints").
//
// vtable order -- this is what the binary actually holds, and `virtual` must be
// declared in this order:
//   [  0] 0x13bec4  CursorObject::~CursorObject
//   [  1] 0x13bee4  CursorObject::~CursorObject
//   [  2] 0x1b7b80  GameObject::BeginDraw
//   [  3] 0x1b7af0  GameObject::EndDraw
//   [  4] 0x1b7b1c  GameObject::MakeParentGraphicsFrame
// Slots 2/3/4 point at GameObject's own implementations, so this class does not
// override them. The authoritative header also declares BeginDraw/EndDraw, but that
// is the hook project intercepting virtual calls; this rebuild has no hooks, and
// adding the overrides would retarget those slots. Update/Draw/Die are not virtual
// in the binary.

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "BaseGamepadControls.h"
#include "CutScene.h"
#include "GameObject.h"
#include "Graphics.h"
#include "Reanimation.h"
#include "ZenGarden.h"

// Neither Board nor LawnApp is named in this file, so both includes were pure
// cycle (CursorObject.h <-> Board.h and CursorObject.h <-> LawnApp.h). They kept
// LawnApp.h's 75 includes and Board.h in every translation unit that reads this
// header. Kept as declarations so the names stay visible to a future reader.
// Sexy\ReconTypes.h already declares LawnApp.
class Board;
class LawnApp;

class CursorObject : public GameObject
{
public:
    virtual ~CursorObject();

    CursorObject();

    void Update();
    void Draw(Sexy::Graphics *theGraphics);
    void Die();

    // Derived member area +0x34..+0x5B (40 bytes); the constructor registers the
    // whole block as one sync block.
    int mSelectedIndex;            // +0x34
    SeedType mType;                // +0x38
    SeedType mImitaterType;        // +0x3C
    CursorType mCursorType;        // +0x40
    // TODO(recon): the authoritative header types this as `CoinID`, but this tree has
    // no CoinID type yet (PvZ\Lawn\Common\ConstEnums.h:1787 only declares
    // `enum CoinID { COINID_NULL };`). Kept as int until that enum is added.
    int mCoinID;                   // +0x44
    PlantID mGlovePlantID;         // +0x48 (the .cpp reads it as 16-bit; see case 3 in Draw)
    PlantID mDuplicatorPlantID;    // +0x4C
    PlantID mCobCannonPlantID;     // +0x50
    int mHammerDownCounter;        // +0x54 (unused by both .cpp files, placeholder only)
    ReanimationID mReanimCursorID; // +0x58
};

// Layout check: 0x34 (GameObject) + 10 four-byte members.
